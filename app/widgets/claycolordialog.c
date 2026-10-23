/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color_dialog module (C) 1998 Austin Donnelly <austin@greenend.org.uk>
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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/clay-palettes.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-color-profile.h"
#include "core/clayimage-colormap.h"
#include "core/claymarshal.h"
#include "core/claypalettemru.h"
#include "core/clayprojection.h"

#include "clayactiongroup.h"
#include "claycolordialog.h"
#include "claycolorhistory.h"
#include "claycolormapselection.h"
#include "claydialogfactory.h"
#include "clayhelp-ids.h"
#include "claywidgets-constructors.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


#define RESPONSE_RESET   1
#define COLOR_AREA_SIZE 20


enum
{
  UPDATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_USER_CONTEXT_AWARE
};

static void   clay_color_dialog_constructed      (GObject            *object);
static void   clay_color_dialog_finalize         (GObject            *object);
static void   clay_color_dialog_set_property     (GObject            *object,
                                                  guint               property_id,
                                                  const GValue       *value,
                                                  GParamSpec         *pspec);
static void   clay_color_dialog_get_property     (GObject            *object,
                                                  guint               property_id,
                                                  GValue             *value,
                                                  GParamSpec         *pspec);

static void   clay_color_dialog_style_updated    (GtkWidget          *widget);

static void   clay_color_dialog_response         (GtkDialog          *dialog,
                                                  gint                response_id);

static void   clay_color_dialog_help_func        (const gchar        *help_id,
                                                  gpointer            help_data);

static void   clay_color_dialog_colormap_clicked (ClayColorDialog    *dialog,
                                                  ClayPaletteEntry   *entry,
                                                  GdkModifierType     state);
static void
        clay_color_dialog_colormap_edit_activate (ClayColorDialog    *dialog);
static void
        clay_color_dialog_colormap_add_activate  (ClayColorDialog    *dialog);

static void   clay_color_dialog_color_changed    (ClayColorSelection *selection,
                                                  ClayColorDialog    *dialog);

static void   clay_color_history_add_clicked     (GtkWidget          *widget,
                                                  ClayColorDialog    *dialog);

static void   clay_color_dialog_image_changed    (ClayContext        *context,
                                                  ClayImage          *image,
                                                  ClayColorDialog    *dialog);
static void   clay_color_dialog_update_format    (ClayImage          *image,
                                                  ClayColorDialog    *dialog);
static void   clay_color_dialog_update_simulation(ClayImage          *image,
                                                  ClayColorDialog    *dialog);
static void   clay_color_dialog_update_base_type (ClayColorDialog    *dialog);
static void   clay_color_dialog_show             (ClayColorDialog    *dialog);
static void   clay_color_dialog_hide             (ClayColorDialog    *dialog);


G_DEFINE_TYPE (ClayColorDialog, clay_color_dialog, CLAY_TYPE_VIEWABLE_DIALOG)

#define parent_class clay_color_dialog_parent_class

static guint color_dialog_signals[LAST_SIGNAL] = { 0, };


static void
clay_color_dialog_class_init (ClayColorDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = clay_color_dialog_constructed;
  object_class->finalize     = clay_color_dialog_finalize;
  object_class->set_property = clay_color_dialog_set_property;
  object_class->get_property = clay_color_dialog_get_property;

  dialog_class->response      = clay_color_dialog_response;

  widget_class->style_updated = clay_color_dialog_style_updated;

  color_dialog_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayColorDialogClass, update),
                  NULL, NULL,
                  clay_marshal_VOID__BOXED_ENUM,
                  G_TYPE_NONE, 2,
                  GEGL_TYPE_COLOR,
                  CLAY_TYPE_COLOR_DIALOG_STATE);

  g_object_class_install_property (object_class, PROP_USER_CONTEXT_AWARE,
                                   g_param_spec_boolean ("user-context-aware",
                                                        NULL, NULL, FALSE,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_color_dialog_init (ClayColorDialog *dialog)
{
  dialog->stack = gtk_stack_new ();
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      dialog->stack, TRUE, TRUE, 0);
  gtk_widget_set_visible (dialog->stack, TRUE);
}

static void
clay_color_dialog_constructed (GObject *object)
{
  ClayColorDialog    *dialog          = CLAY_COLOR_DIALOG (object);
  ClayViewableDialog *viewable_dialog = CLAY_VIEWABLE_DIALOG (object);
  GtkWidget          *hbox;
  GtkWidget          *history;
  GtkWidget          *button;
  GtkWidget          *image;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /** Tab: colormap selection. **/
  dialog->colormap_selection = clay_colormap_selection_new (viewable_dialog->context->clay->user_context);
  gtk_stack_add_named (GTK_STACK (dialog->stack),
                       dialog->colormap_selection, "colormap");
  g_signal_connect_swapped (dialog->colormap_selection, "color-clicked",
                            G_CALLBACK (clay_color_dialog_colormap_clicked),
                            dialog);
  g_signal_connect_swapped (dialog->colormap_selection, "color-activated",
                            G_CALLBACK (clay_color_dialog_colormap_edit_activate),
                            dialog);
  gtk_widget_set_visible (dialog->colormap_selection, TRUE);

  /* Edit color button */
  button = clay_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), FALSE);

  image = gtk_image_new_from_icon_name (CLAY_ICON_EDIT, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_visible (image, TRUE);

  /* XXX: I use the same icon, tooltip and help id as in
   * colormap-actions.c. I wanted to just load these strings from
   * the action, but the group is only registered when the Colormap
   * dockable is opened.
   */
  clay_help_set_help_data_with_markup (button,
                                       C_("colormap-action", "Edit this color"),
                                       CLAY_HELP_INDEXED_PALETTE_EDIT);
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (clay_color_dialog_colormap_edit_activate),
                            dialog);
  gtk_widget_set_visible (button, TRUE);
  gtk_box_pack_start (GTK_BOX (CLAY_COLORMAP_SELECTION (dialog->colormap_selection)->right_vbox), button,
                      FALSE, FALSE, 0);

  /* Add Color button */
  button = clay_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), FALSE);

  image = gtk_image_new_from_icon_name (CLAY_ICON_LIST_ADD, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_visible (image, TRUE);

  clay_help_set_help_data_with_markup (button,
                                       C_("colormap-action", "Add current foreground color"),
                                       CLAY_HELP_INDEXED_PALETTE_ADD);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (clay_color_dialog_colormap_add_activate),
                            dialog);
  gtk_widget_set_visible (button, TRUE);
  gtk_box_pack_start (GTK_BOX (CLAY_COLORMAP_SELECTION (dialog->colormap_selection)->right_vbox), button,
                      FALSE, FALSE, 0);

  /** Tab: color selection. **/
  dialog->selection = clay_color_selection_new ();
  gtk_container_set_border_width (GTK_CONTAINER (dialog->selection), 12);
  gtk_stack_add_named (GTK_STACK (dialog->stack), dialog->selection, "color");
  gtk_widget_set_visible (dialog->selection, TRUE);

  g_signal_connect (dialog->selection, "color-changed",
                    G_CALLBACK (clay_color_dialog_color_changed),
                    dialog);

  /* Color history box. */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_end (GTK_BOX (clay_color_selection_get_right_vbox (CLAY_COLOR_SELECTION (dialog->selection))),
                    hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  /* Button for adding to color history. */
  button = clay_icon_button_new (CLAY_ICON_LIST_ADD, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (button), FALSE, FALSE, 0);
  clay_help_set_help_data (button,
                           _("Add the current color to the color history"),
                           NULL);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (clay_color_history_add_clicked),
                    dialog);

  /* Color history table. */
  history = clay_color_history_new (viewable_dialog->context, 12);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (history), TRUE, TRUE, 0);
  gtk_widget_set_visible (GTK_WIDGET (history), TRUE);

  g_signal_connect_swapped (history, "color-selected",
                            G_CALLBACK (clay_color_selection_set_color),
                            dialog->selection);

  g_signal_connect (dialog, "show",
                    G_CALLBACK (clay_color_dialog_show),
                    NULL);
  g_signal_connect (dialog, "hide",
                    G_CALLBACK (clay_color_dialog_hide),
                    NULL);
  clay_color_dialog_show (dialog);
}

static void
clay_color_dialog_finalize (GObject *object)
{
  ClayColorDialog    *dialog          = CLAY_COLOR_DIALOG (object);
  ClayViewableDialog *viewable_dialog = CLAY_VIEWABLE_DIALOG (dialog);

  g_clear_weak_pointer (&dialog->active_image);

  if (dialog->user_context_aware && viewable_dialog->context)
    {
      ClayContext *user_context = viewable_dialog->context->clay->user_context;

      g_signal_handlers_disconnect_by_func (user_context,
                                            G_CALLBACK (clay_color_dialog_image_changed),
                                            dialog);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_color_dialog_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayColorDialog *dialog = CLAY_COLOR_DIALOG (object);

  switch (property_id)
    {
    case PROP_USER_CONTEXT_AWARE:
      dialog->user_context_aware = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_dialog_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayColorDialog *dialog = CLAY_COLOR_DIALOG (object);

  switch (property_id)
    {
    case PROP_USER_CONTEXT_AWARE:
      g_value_set_boolean (value, dialog->user_context_aware);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_dialog_style_updated (GtkWidget *widget)
{
  ClayColorDialog   *dialog = CLAY_COLOR_DIALOG (widget);
  ClayColorNotebook *notebook;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  notebook =
    CLAY_COLOR_NOTEBOOK (clay_color_selection_get_notebook (CLAY_COLOR_SELECTION (dialog->selection)));
  GTK_WIDGET_GET_CLASS (notebook)->style_updated (GTK_WIDGET (notebook));
}

static void
clay_color_dialog_response (GtkDialog *gtk_dialog,
                            gint       response_id)
{
  ClayColorDialog       *dialog          = CLAY_COLOR_DIALOG (gtk_dialog);
  ClayViewableDialog    *viewable_dialog = CLAY_VIEWABLE_DIALOG (dialog);
  ClayImage             *image           = NULL;
  ClayColormapSelection *colormap_selection;
  gint                   col_index;
  GeglColor             *color           = NULL;

  colormap_selection = CLAY_COLORMAP_SELECTION (dialog->colormap_selection);
  col_index = clay_colormap_selection_get_index (colormap_selection, NULL);

  if (dialog->colormap_editing && viewable_dialog->context)
    {
      ClayContext *user_context = viewable_dialog->context->clay->user_context;

      image = clay_context_get_image (user_context);
    }

  switch (response_id)
    {
    case RESPONSE_RESET:
      clay_color_selection_reset (CLAY_COLOR_SELECTION (dialog->selection));
      break;

    case GTK_RESPONSE_OK:
      color = clay_color_selection_get_color (CLAY_COLOR_SELECTION (dialog->selection));

      if (dialog->colormap_editing && image)
        {
          GeglColor *old_color;

          dialog->colormap_editing = FALSE;

          /* Restore old color for undo */
          old_color = clay_color_selection_get_old_color (CLAY_COLOR_SELECTION (dialog->selection));

          if (clay_image_get_base_type (image) == CLAY_INDEXED)
            {
              clay_image_set_colormap_entry (image, col_index, old_color, FALSE);
              clay_image_set_colormap_entry (image, col_index, color, TRUE);
              clay_image_flush (image);
            }

          gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "colormap");
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         color, CLAY_COLOR_DIALOG_UPDATE);

          g_object_unref (old_color);
        }
      else
        {
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         color, CLAY_COLOR_DIALOG_OK);
        }

      g_object_unref (color);
      break;

    default:
      color = clay_color_selection_get_old_color (CLAY_COLOR_SELECTION (dialog->selection));

      if (dialog->colormap_editing && image)
        {
          dialog->colormap_editing = FALSE;

          if (clay_image_get_base_type (image) == CLAY_INDEXED)
            {
              clay_image_set_colormap_entry (image, col_index, color, FALSE);
              clay_projection_flush (clay_image_get_projection (image));
            }

          gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "colormap");
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         color, CLAY_COLOR_DIALOG_UPDATE);
        }
      else
        {
          g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                         color, CLAY_COLOR_DIALOG_CANCEL);
        }

      g_object_unref (color);
      break;
    }
}


/*  public functions  */

GtkWidget *
clay_color_dialog_new (ClayViewable      *viewable,
                       ClayContext       *context,
                       gboolean           user_context_aware,
                       const gchar       *title,
                       const gchar       *icon_name,
                       const gchar       *desc,
                       GtkWidget         *parent,
                       ClayDialogFactory *dialog_factory,
                       const gchar       *dialog_identifier,
                       GeglColor         *color,
                       gboolean           wants_updates,
                       gboolean           show_alpha)
{
  ClayColorDialog *dialog;
  const gchar     *role;
  gboolean         use_header_bar;

  g_return_val_if_fail (viewable == NULL || CLAY_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        CLAY_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (dialog_factory == NULL || dialog_identifier != NULL,
                        NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  role = dialog_identifier ? dialog_identifier : "clay-color-selector";

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (CLAY_TYPE_COLOR_DIALOG,
                         "title",              title,
                         "role",               role,
                         "help-func",          clay_color_dialog_help_func,
                         "help-id",            CLAY_HELP_COLOR_DIALOG,
                         "icon-name",          icon_name,
                         "description",        desc,
                         "context",            context,
                         "user-context-aware", user_context_aware,
                         "parent",             gtk_widget_get_toplevel (parent),
                         "use-header-bar",     use_header_bar,
                         NULL);

  clay_dialog_add_buttons (CLAY_DIALOG (dialog),

                           _("_Reset"),  RESPONSE_RESET,
                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           _("_OK"),     GTK_RESPONSE_OK,

                           NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  if (viewable)
    {
      clay_viewable_dialog_set_viewables (CLAY_VIEWABLE_DIALOG (dialog),
                                          g_list_prepend (NULL, viewable), context);
    }
  else
    {
      GtkWidget *parent;

      parent = gtk_widget_get_parent (CLAY_VIEWABLE_DIALOG (dialog)->icon);
      parent = gtk_widget_get_parent (parent);

      gtk_widget_set_visible (parent, FALSE);
    }

  dialog->wants_updates = wants_updates;

  if (dialog_factory)
    {
      clay_dialog_factory_add_foreign (dialog_factory, dialog_identifier,
                                       GTK_WIDGET (dialog),
                                       clay_widget_get_monitor (parent));
    }

  clay_color_selection_set_show_alpha (CLAY_COLOR_SELECTION (dialog->selection),
                                       show_alpha);

  g_object_set_data (G_OBJECT (context->clay->config->color_management),
                     "clay-context", context);

  clay_color_selection_set_config (CLAY_COLOR_SELECTION (dialog->selection),
                                   context->clay->config->color_management);

  g_object_set_data (G_OBJECT (context->clay->config->color_management),
                     "clay-context", NULL);

  clay_color_selection_set_color (CLAY_COLOR_SELECTION (dialog->selection),
                                  color);
  clay_color_selection_set_old_color (CLAY_COLOR_SELECTION (dialog->selection),
                                      color);

  return GTK_WIDGET (dialog);
}

void
clay_color_dialog_set_color (ClayColorDialog *dialog,
                             GeglColor       *color)
{
  g_return_if_fail (CLAY_IS_COLOR_DIALOG (dialog));
  g_return_if_fail (GEGL_IS_COLOR (color));

  g_signal_handlers_block_by_func (dialog->selection,
                                   clay_color_dialog_color_changed,
                                   dialog);

  clay_color_selection_set_color (CLAY_COLOR_SELECTION (dialog->selection),
                                  color);
  clay_color_selection_set_old_color (CLAY_COLOR_SELECTION (dialog->selection),
                                      color);
  if (! dialog->user_context_aware)
    clay_color_selection_set_format (CLAY_COLOR_SELECTION (dialog->selection),
                                     gegl_color_get_format (color));

  if (dialog->active_image &&
      clay_image_get_base_type (dialog->active_image) == CLAY_INDEXED)
    {
      ClayColormapSelection *colormap_selection;
      gint                   col_index;

      colormap_selection = CLAY_COLORMAP_SELECTION (dialog->colormap_selection);
      col_index = clay_colormap_selection_get_index (colormap_selection, color);

      clay_colormap_selection_set_index (colormap_selection, col_index, NULL);
    }

  g_signal_handlers_unblock_by_func (dialog->selection,
                                     clay_color_dialog_color_changed,
                                     dialog);
}

GeglColor *
clay_color_dialog_get_color (ClayColorDialog *dialog)
{
  g_return_val_if_fail (CLAY_IS_COLOR_DIALOG (dialog), NULL);

  return clay_color_selection_get_color (CLAY_COLOR_SELECTION (dialog->selection));
}


/*  private functions  */

static void
clay_color_dialog_help_func (const gchar *help_id,
                             gpointer     help_data)
{
  ClayColorDialog   *dialog = CLAY_COLOR_DIALOG (help_data);
  ClayColorNotebook *notebook;
  ClayColorSelector *current;

  notebook =
    CLAY_COLOR_NOTEBOOK (clay_color_selection_get_notebook (CLAY_COLOR_SELECTION (dialog->selection)));

  current = clay_color_notebook_get_current_selector (notebook);

  help_id = CLAY_COLOR_SELECTOR_GET_CLASS (current)->help_id;

  clay_standard_help_func (help_id, NULL);
}

static void
clay_color_dialog_colormap_clicked (ClayColorDialog  *dialog,
                                    ClayPaletteEntry *entry,
                                    GdkModifierType   state)
{
  clay_color_dialog_set_color (dialog, entry->color);

  if (dialog->wants_updates)
    g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                   entry->color, CLAY_COLOR_DIALOG_UPDATE);
}

static void
clay_color_dialog_colormap_edit_activate (ClayColorDialog *dialog)
{
  ClayColormapSelection *colormap_selection;
  GeglColor             *color;
  gint                   col_index;

  dialog->colormap_editing = TRUE;

  colormap_selection = CLAY_COLORMAP_SELECTION (dialog->colormap_selection);
  col_index = clay_colormap_selection_get_index (colormap_selection, NULL);
  color = clay_image_get_colormap_entry (dialog->active_image, col_index);
  clay_color_dialog_set_color (dialog, color);

  gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "color");
}

static void
clay_color_dialog_colormap_add_activate (ClayColorDialog *dialog)
{
  ClayViewableDialog *viewable_dialog = CLAY_VIEWABLE_DIALOG (dialog);

  if (dialog->active_image &&
      clay_palette_get_n_colors (clay_image_get_colormap_palette (dialog->active_image)) < 256 &&
      viewable_dialog->context)
    {
      ClayContext *user_context = viewable_dialog->context->clay->user_context;
      GeglColor   *color        = clay_context_get_foreground (user_context);

      clay_image_add_colormap_entry (dialog->active_image, color);
      clay_image_flush (dialog->active_image);
    }
}

static void
clay_color_dialog_color_changed (ClayColorSelection *selection,
                                 ClayColorDialog    *dialog)
{
  ClayViewableDialog *viewable_dialog = CLAY_VIEWABLE_DIALOG (dialog);
  GeglColor          *color;

  color = clay_color_selection_get_color (selection);

  if (dialog->colormap_editing && viewable_dialog->context)
    {
      ClayContext *user_context = viewable_dialog->context->clay->user_context;
      ClayImage   *image;

      image = clay_context_get_image (user_context);
      if (image)
        {
          ClayColormapSelection *colormap_selection;
          gboolean               push_undo = FALSE;
          gint                   col_index;

          colormap_selection = CLAY_COLORMAP_SELECTION (dialog->colormap_selection);
          col_index = clay_colormap_selection_get_index (colormap_selection, NULL);
          if (push_undo)
            {
              GeglColor *old_color;

              old_color = clay_color_selection_get_old_color (CLAY_COLOR_SELECTION (dialog->selection));

              /* Restore old color for undo */
              clay_image_set_colormap_entry (image, col_index, old_color, FALSE);

              g_object_unref (old_color);
            }

          if (clay_image_get_base_type (image) == CLAY_INDEXED)
            clay_image_set_colormap_entry (image, col_index, color, push_undo);

          if (push_undo)
            clay_image_flush (image);
          else
            clay_projection_flush (clay_image_get_projection (image));
        }
    }

  if (dialog->wants_updates)
    g_signal_emit (dialog, color_dialog_signals[UPDATE], 0,
                   color, CLAY_COLOR_DIALOG_UPDATE);

  g_object_unref (color);
}


/* History-adding button callback */

static void
clay_color_history_add_clicked (GtkWidget       *widget,
                                ClayColorDialog *dialog)
{
  ClayViewableDialog *viewable_dialog = CLAY_VIEWABLE_DIALOG (dialog);
  ClayPalette        *history;
  GeglColor          *color;

  history = clay_palettes_get_color_history (viewable_dialog->context->clay);
  color   = clay_color_selection_get_color (CLAY_COLOR_SELECTION (dialog->selection));
  clay_palette_mru_add (CLAY_PALETTE_MRU (history), color);
}

/* Context-related callbacks */

static void
clay_color_dialog_image_changed (ClayContext     *context,
                                 ClayImage       *image,
                                 ClayColorDialog *dialog)
{
  if (dialog->active_image != image)
    {
      if (dialog->active_image)
        {
          g_signal_handlers_disconnect_by_func (dialog->active_image,
                                                G_CALLBACK (clay_color_dialog_update_base_type),
                                                dialog);
          g_signal_handlers_disconnect_by_func (dialog->active_image,
                                                clay_color_dialog_update_simulation,
                                                dialog);
        }

      g_set_weak_pointer (&dialog->active_image, image);

      if (image)
        {
          g_signal_connect_object (image, "notify::base-type",
                                   G_CALLBACK (clay_color_dialog_update_base_type),
                                   dialog, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "profile-changed",
                                   G_CALLBACK (clay_color_dialog_update_format),
                                   dialog, 0);
          g_signal_connect_object (image, "simulation-profile-changed",
                                   G_CALLBACK (clay_color_dialog_update_simulation),
                                   dialog, 0);
          g_signal_connect_object (image, "simulation-intent-changed",
                                   G_CALLBACK (clay_color_dialog_update_simulation),
                                   dialog, 0);
          g_signal_connect_object (image, "simulation-bpc-changed",
                                   G_CALLBACK (clay_color_dialog_update_simulation),
                                   dialog, 0);

          clay_color_dialog_update_simulation (image, dialog);
        }

      clay_color_dialog_update_base_type (dialog);
      clay_color_dialog_update_format (image, dialog);
    }
}

static void
clay_color_dialog_update_format (ClayImage       *image,
                                 ClayColorDialog *dialog)
{
  const Babl *format = NULL;

  g_return_if_fail (CLAY_IS_COLOR_DIALOG (dialog));

  if (image)
    format = clay_image_get_layer_format (image, FALSE);

  clay_color_selection_set_format (CLAY_COLOR_SELECTION (dialog->selection), format);
}

static void
clay_color_dialog_update_simulation (ClayImage       *image,
                                     ClayColorDialog *dialog)
{
  ClayColorProfile         *profile = NULL;
  ClayColorRenderingIntent  intent  = CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  gboolean                  bpc     = FALSE;

  g_return_if_fail (CLAY_IS_COLOR_DIALOG (dialog));

  if (image)
    {
      profile = clay_image_get_simulation_profile (image);
      intent  = clay_image_get_simulation_intent (image);
      bpc     = clay_image_get_simulation_bpc (image);
    }

  clay_color_selection_set_simulation (CLAY_COLOR_SELECTION (dialog->selection),
                                       profile, intent, bpc);
}

static void
clay_color_dialog_update_base_type (ClayColorDialog *dialog)
{
  if (dialog->active_image &&
      clay_image_get_base_type (dialog->active_image) == CLAY_INDEXED)
    {
      ClayColormapSelection *colormap_selection;
      GeglColor             *color;
      gint                   col_index;

      gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "colormap");

      /* Update selected color in palette */
      colormap_selection = CLAY_COLORMAP_SELECTION (dialog->colormap_selection);

      color = clay_color_dialog_get_color (dialog);
      col_index = clay_colormap_selection_get_index (colormap_selection, color);
      g_object_unref (color);

      clay_colormap_selection_set_index (colormap_selection, col_index, NULL);
    }
  else
    {
      gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "color");
    }
}

static void
clay_color_dialog_show (ClayColorDialog *dialog)
{
  ClayViewableDialog *viewable_dialog = CLAY_VIEWABLE_DIALOG (dialog);

  dialog->colormap_editing = FALSE;

  if (dialog->user_context_aware && viewable_dialog->context)
    {
      ClayContext *user_context = viewable_dialog->context->clay->user_context;
      ClayImage   *image        = clay_context_get_image (user_context);

      g_signal_connect_object (user_context, "image-changed",
                               G_CALLBACK (clay_color_dialog_image_changed),
                               dialog, 0);
      g_signal_connect_object (viewable_dialog->context->clay->config,
                               "notify::theme",
                               G_CALLBACK (clay_color_dialog_style_updated),
                               dialog, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (viewable_dialog->context->clay->config,
                               "notify::override-theme-icon-size",
                               G_CALLBACK (clay_color_dialog_style_updated),
                               dialog, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (viewable_dialog->context->clay->config,
                               "notify::custom-icon-size",
                               G_CALLBACK (clay_color_dialog_style_updated),
                               dialog, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

      clay_color_dialog_image_changed (viewable_dialog->context,
                                       image, dialog);
      clay_color_dialog_update_base_type (dialog);
      clay_color_dialog_style_updated (GTK_WIDGET (dialog));
    }
  else
    {
      gtk_stack_set_visible_child_name (GTK_STACK (dialog->stack), "color");
    }
}

static void
clay_color_dialog_hide (ClayColorDialog *dialog)
{
  ClayViewableDialog *viewable_dialog = CLAY_VIEWABLE_DIALOG (dialog);

  if (dialog->user_context_aware && viewable_dialog->context)
    {
      ClayContext *user_context = viewable_dialog->context->clay->user_context;
      g_signal_handlers_disconnect_by_func (user_context,
                                            G_CALLBACK (clay_color_dialog_image_changed),
                                            dialog);
      g_signal_handlers_disconnect_by_func (viewable_dialog->context->clay->config,
                                            G_CALLBACK (clay_color_dialog_style_updated),
                                            dialog);
    }
}
