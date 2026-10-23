/* Lighting Effects - A plug-in for Clay
 *
 * Dialog creation and updaters, callbacks and event-handlers
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
#include <errno.h>

#include <glib/gstdio.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "lighting-ui.h"
#include "lighting-main.h"
#include "lighting-icons.h"
#include "lighting-image.h"
#include "lighting-apply.h"
#include "lighting-preview.h"

#include "libclay/stdplugins-intl.h"


extern LightingValues mapvals;

static GtkWidget   *appwin            = NULL;

GtkWidget *previewarea                = NULL;

GtkWidget *spin_pos_x                 = NULL;
GtkWidget *spin_pos_y                 = NULL;
GtkWidget *spin_pos_z                 = NULL;
GtkWidget *spin_dir_x                 = NULL;
GtkWidget *spin_dir_y                 = NULL;
GtkWidget *spin_dir_z                 = NULL;

static gchar     *lighting_effects_path = NULL;

static void     update_preview         (ClayProcedureConfig *config);

static void     save_lighting_preset   (GtkWidget           *widget,
                                        gpointer             data);
static void     save_preset_response   (GtkNativeDialog     *chooser,
                                        gint                 response_id,
                                        gpointer             data);
static void     load_lighting_preset   (GtkWidget           *widget,
                                        gpointer             data);
static void     load_preset_response   (GtkNativeDialog     *chooser,
                                        gint                 response_id,
                                        gpointer             data);
static void     light_source_changed   (GtkWidget           *widget,
                                        gpointer             data);
static void     apply_settings         (ClayProcedureConfig *config);
static void     isolate_selected_light (ClayProcedureConfig *config);

/*****************************************/
/* Main window light type menu callback. */
/*****************************************/

static void
apply_settings (ClayProcedureConfig *config)
{
  gint k       = mapvals.light_selected;
  gchar *pos_x = g_strdup_printf (("light-position-x-%d"), k + 1);
  gchar *pos_y = g_strdup_printf (("light-position-y-%d"), k + 1);
  gchar *pos_z = g_strdup_printf (("light-position-z-%d"), k + 1);
  gchar *dir_x = g_strdup_printf (("light-direction-x-%d"), k + 1);
  gchar *dir_y = g_strdup_printf (("light-direction-y-%d"), k + 1);
  gchar *dir_z = g_strdup_printf (("light-direction-z-%d"), k + 1);

  switch (mapvals.lightsource[k].type)
    {
    case NO_LIGHT:
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), pos_x,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), pos_y,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), pos_z,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), dir_x,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), dir_y,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), dir_z,
                                           FALSE, NULL, NULL, FALSE);
      break;
    case POINT_LIGHT:
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), pos_x,
                                           TRUE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), pos_y,
                                           TRUE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), pos_z,
                                           TRUE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), dir_x,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), dir_y,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), dir_z,
                                           FALSE, NULL, NULL, FALSE);
      break;
    case DIRECTIONAL_LIGHT:
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), pos_x,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), pos_y,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), pos_z,
                                           FALSE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), dir_x,
                                           TRUE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), dir_y,
                                           TRUE, NULL, NULL, FALSE);
      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (appwin), dir_z,
                                           TRUE, NULL, NULL, FALSE);
      break;
    default:
      break;
    }

  g_free (pos_x);
  g_free (pos_y);
  g_free (pos_z);
  g_free (dir_x);
  g_free (dir_y);
  g_free (dir_z);
}

/******************************************/
/* Main window "Preview!" button callback */
/******************************************/

static void
preview_callback (GtkWidget *widget)
{
  preview_compute ();
  gtk_widget_queue_draw (previewarea);
}

static void
update_preview (ClayProcedureConfig *config)
{
  copy_from_config (config);
  isolate_selected_light (config);

  apply_settings (config);

  preview_compute ();
  gtk_widget_queue_draw (previewarea);
}

/********************************/
/* Create and show main dialog. */
/********************************/

gboolean
main_dialog (ClayProcedure       *procedure,
             ClayProcedureConfig *config,
             ClayDrawable        *drawable)
{
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *combo;
  GtkWidget *image;
  gchar     *path;
  gboolean   run = FALSE;

  /*
  GtkWidget *image;
  */

  clay_ui_init (PLUG_IN_BINARY);

  path = clay_clayrc_query ("lighting-effects-path");
  if (path)
    {
      lighting_effects_path = g_filename_from_utf8 (path, -1, NULL, NULL, NULL);
      g_free (path);
    }

  lighting_icons_init ();

  appwin = clay_procedure_dialog_new (procedure,
                                      CLAY_PROCEDURE_CONFIG (config),
                                      _("Lighting Effects"));

  /* Create the Preview */
  /* ================== */

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_visible (vbox, TRUE);

  /* Add preview widget and various buttons to the first part */
  /* ======================================================== */

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  gtk_widget_realize (appwin);

  previewarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (previewarea, PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (previewarea, (GDK_EXPOSURE_MASK |
                                       GDK_BUTTON1_MOTION_MASK |
                                       GDK_BUTTON_PRESS_MASK |
                                       GDK_BUTTON_RELEASE_MASK));
  g_signal_connect (previewarea, "event",
                    G_CALLBACK (preview_events),
                    previewarea);
  g_signal_connect (previewarea, "draw",
                    G_CALLBACK (preview_draw),
                    previewarea);
  gtk_container_add (GTK_CONTAINER (frame), previewarea);
  gtk_widget_set_visible (previewarea, TRUE);

  /* create preview options, frame and vbox */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  button = gtk_button_new_with_mnemonic (_("_Update"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (preview_callback),
                    NULL);
  gtk_widget_set_visible (button, TRUE);

  clay_help_set_help_data (button, _("Recompute preview image"), NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("I_nteractive"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mapvals.interactive_preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (clay_toggle_button_update),
                    &mapvals.interactive_preview);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (interactive_preview_callback),
                    NULL);

  gtk_widget_set_visible (toggle, TRUE);

  clay_help_set_help_data (toggle,
                           _("Enable/disable real time preview of changes"),
                           NULL);

  /* ClayProcedureDialog begins here */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "options-tab", _("Op_tions"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "light-tab", _("_Light"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "material-tab", _("_Material"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "bump-map-tab", _("_Bump Map"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "environment-map-tab", _("_Environment Map"),
                                   FALSE, TRUE);

  /* Options tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "general-options", _("General Options"),
                                   FALSE, FALSE);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "distance", 1.0);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "general-box",
                                  "transparent-background",
                                  "new-image",
                                  "antialiasing",
                                  "distance",
                                  NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "options-frame",
                                    "general-options", FALSE,
                                    "general-box");
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "options-box",
                                  "options-frame", NULL);

  g_signal_connect (config, "notify::transparent-background",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::new-image",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::antialiasing",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::distance",
                    G_CALLBACK (update_preview),
                    config);

  /* Light tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "light-label", _("Light Settings"),
                                   FALSE, FALSE);

  for (gint i = 1; i <= 6; i++)
    {
      gchar *temp_box;
      gchar *temp_label;
      gchar *temp_field_1;
      gchar *temp_field_2;
      gchar *temp_field_3;
      gchar *temp_notify;

      temp_label = g_strdup_printf ("light-color-label-%d", i);
      clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                       temp_label, _("Color"),
                                       FALSE, FALSE);
      g_free (temp_label);
      temp_label = g_strdup_printf ("light-position-label-%d", i);
      clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                       temp_label, _("Position"),
                                       FALSE, FALSE);
      g_free (temp_label);
      temp_label = g_strdup_printf ("light-direction-label-%d", i);
      clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                       temp_label, _("Direction"),
                                       FALSE, FALSE);
      g_free (temp_label);

      temp_box = g_strdup_printf ("light-color-vbox-%d", i);
      temp_label = g_strdup_printf ("light-color-label-%d", i);
      temp_field_1 = g_strdup_printf ("light-type-%d", i);
      temp_field_2 = g_strdup_printf ("light-color-%d", i);
      temp_field_3 = g_strdup_printf ("light-intensity-%d", i);
      clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), temp_box,
                                      temp_label,
                                      temp_field_1,
                                      temp_field_2,
                                      temp_field_3,
                                      NULL);

      temp_notify = g_strdup_printf ("notify::%s", temp_field_1);
      g_signal_connect (config, temp_notify,
                        G_CALLBACK (update_preview),
                        config);
      g_free (temp_notify);
      temp_notify = g_strdup_printf ("notify::%s", temp_field_2);
      g_signal_connect (config, temp_notify,
                        G_CALLBACK (update_preview),
                        config);
      g_free (temp_notify);
      temp_notify = g_strdup_printf ("notify::%s", temp_field_3);
      g_signal_connect (config, temp_notify,
                        G_CALLBACK (update_preview),
                        config);
      g_free (temp_notify);

      g_free (temp_box);
      g_free (temp_label);
      g_free (temp_field_1);
      g_free (temp_field_2);
      g_free (temp_field_3);

      temp_box = g_strdup_printf ("light-position-vbox-%d", i);
      temp_label = g_strdup_printf ("light-position-label-%d", i);
      temp_field_1 = g_strdup_printf ("light-position-x-%d", i);
      temp_field_2 = g_strdup_printf ("light-position-y-%d", i);
      temp_field_3 = g_strdup_printf ("light-position-z-%d", i);
      clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), temp_box,
                                      temp_label,
                                      temp_field_1,
                                      temp_field_2,
                                      temp_field_3,
                                      NULL);

      temp_notify = g_strdup_printf ("notify::%s", temp_field_1);
      g_signal_connect (config, temp_notify,
                        G_CALLBACK (update_preview),
                        config);
      g_free (temp_notify);
      temp_notify = g_strdup_printf ("notify::%s", temp_field_2);
      g_signal_connect (config, temp_notify,
                        G_CALLBACK (update_preview),
                        config);
      g_free (temp_notify);
      temp_notify = g_strdup_printf ("notify::%s", temp_field_3);
      g_signal_connect (config, temp_notify,
                        G_CALLBACK (update_preview),
                        config);
      g_free (temp_notify);

      g_free (temp_box);
      g_free (temp_label);
      g_free (temp_field_1);
      g_free (temp_field_2);
      g_free (temp_field_3);

      temp_box = g_strdup_printf ("light-direction-vbox-%d", i);
      temp_label = g_strdup_printf ("light-direction-label-%d", i);
      temp_field_1 = g_strdup_printf ("light-direction-x-%d", i);
      temp_field_2 = g_strdup_printf ("light-direction-y-%d", i);
      temp_field_3 = g_strdup_printf ("light-direction-z-%d", i);
      clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), temp_box,
                                      temp_label,
                                      temp_field_1,
                                      temp_field_2,
                                      temp_field_3,
                                      NULL);

      temp_notify = g_strdup_printf ("notify::%s", temp_field_1);
      g_signal_connect (config, temp_notify,
                        G_CALLBACK (update_preview),
                        config);
      g_free (temp_notify);
      temp_notify = g_strdup_printf ("notify::%s", temp_field_2);
      g_signal_connect (config, temp_notify,
                        G_CALLBACK (update_preview),
                        config);
      g_free (temp_notify);
      temp_notify = g_strdup_printf ("notify::%s", temp_field_3);
      g_signal_connect (config, temp_notify,
                        G_CALLBACK (update_preview),
                        config);
      g_free (temp_notify);

      g_free (temp_box);
      g_free (temp_label);
      g_free (temp_field_1);
      g_free (temp_field_2);
      g_free (temp_field_3);

      temp_box = g_strdup_printf ("light-hbox-%d", i);
      temp_field_1 = g_strdup_printf ("light-color-vbox-%d", i);
      temp_field_2 = g_strdup_printf ("light-position-vbox-%d", i);
      temp_field_3 = g_strdup_printf ("light-direction-vbox-%d", i);
      hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), temp_box,
                                             temp_field_1,
                                             temp_field_2,
                                             temp_field_3,
                                             NULL);
      g_free (temp_box);
      g_free (temp_field_1);
      g_free (temp_field_2);
      g_free (temp_field_3);
      gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                      GTK_ORIENTATION_HORIZONTAL);
    }

  combo = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (appwin),
                                            "which-light", G_TYPE_NONE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "light-settings-box",
                                  "which-light",
                                  "light-hbox-1",
                                  "light-hbox-2",
                                  "light-hbox-3",
                                  "light-hbox-4",
                                  "light-hbox-5",
                                  "light-hbox-6",
                                  "isolate",
                                  NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "light-frame",
                                    "light-label", FALSE,
                                    "light-settings-box");

  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "preset-label", _("Lighting presets: "),
                                   FALSE, FALSE);
  hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "preset-hbox",
                                         "preset-label", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_halign (hbox, GTK_ALIGN_END);

  button = gtk_button_new_with_mnemonic (_("_Save"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (save_lighting_preset),
                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);
  button = gtk_button_new_with_mnemonic (_("_Open"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (load_lighting_preset),
                    NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "light-box",
                                  "light-frame", "preset-hbox", NULL);

  g_signal_connect (config, "notify::which-light",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::isolate",
                    G_CALLBACK (update_preview),
                    config);

  /* Material Tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "material-label", _("Material Properties"),
                                   FALSE, FALSE);

  hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "glowing-hbox",
                                         "ambient-intensity",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  image = gtk_image_new_from_icon_name (LIGHTING_INTENSITY_AMBIENT_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (hbox), image, 0);
  gtk_widget_set_visible (image, TRUE);
  image = gtk_image_new_from_icon_name (LIGHTING_INTENSITY_AMBIENT_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);

  hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "bright-hbox",
                                         "diffuse-intensity",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  image = gtk_image_new_from_icon_name (LIGHTING_INTENSITY_DIFFUSE_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);
  gtk_box_reorder_child (GTK_BOX (hbox), image, 0);
  image = gtk_image_new_from_icon_name (LIGHTING_INTENSITY_DIFFUSE_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);


  hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "diffuse-reflect-hbox",
                                         "diffuse-reflectivity",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  image = gtk_image_new_from_icon_name (LIGHTING_REFLECTIVITY_DIFFUSE_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);
  gtk_box_reorder_child (GTK_BOX (hbox), image, 0);
  image = gtk_image_new_from_icon_name (LIGHTING_REFLECTIVITY_DIFFUSE_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);

  hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "shiny-hbox",
                                         "specular-reflectivity",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  image = gtk_image_new_from_icon_name (LIGHTING_REFLECTIVITY_SPECULAR_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);
  gtk_box_reorder_child (GTK_BOX (hbox), image, 0);
  image = gtk_image_new_from_icon_name (LIGHTING_REFLECTIVITY_SPECULAR_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);

  hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "polished-hbox",
                                         "highlight",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  image = gtk_image_new_from_icon_name (LIGHTING_REFLECTIVITY_HIGHLIGHT_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);
  gtk_box_reorder_child (GTK_BOX (hbox), image, 0);
  image = gtk_image_new_from_icon_name (LIGHTING_REFLECTIVITY_HIGHLIGHT_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_set_visible (image, TRUE);

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "material-values-box",
                                  "glowing-hbox",
                                  "bright-hbox",
                                  "diffuse-reflect-hbox",
                                  "shiny-hbox",
                                  "polished-hbox",
                                  "metallic",
                                  NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "material-frame",
                                    "material-label", FALSE,
                                    "material-values-box");
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "material-box",
                                  "material-frame", NULL);

  g_signal_connect (config, "notify::ambient-intensity",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::diffuse-intensity",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::diffuse-reflectivity",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::specular-reflectivity",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::highlight",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::metallic",
                    G_CALLBACK (update_preview),
                    config);

  /* Bump Map Tab */
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "bump-map-options-box",
                                  "bump-drawable",
                                  "bumpmap-type",
                                  "bumpmap-max-height",
                                  NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "bump-map-frame",
                                    "do-bumpmap", FALSE,
                                    "bump-map-options-box");
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "bump-map-box",
                                  "bump-map-frame",
                                  NULL);

  g_signal_connect (config, "notify::do-bumpmap",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::bump-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::bumpmap-type",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::bumpmap-max-height",
                    G_CALLBACK (update_preview),
                    config);

  /* Environment Map Tab */
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "environment-map-frame",
                                    "do-envmap", FALSE,
                                    "env-drawable");
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "environment-map-box",
                                  "environment-map-frame",
                                  NULL);

  g_signal_connect (config, "notify::do-envmap",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::env-drawable",
                    G_CALLBACK (update_preview),
                    config);

  /* Create Notebook */
  clay_procedure_dialog_fill_notebook (CLAY_PROCEDURE_DIALOG (appwin),
                                       "main-notebook",
                                       "options-tab", "options-box",
                                       "light-tab", "light-box",
                                       "material-tab", "material-box",
                                       "bump-map-tab", "bump-map-box",
                                       "environment-map-tab", "environment-map-box",
                                       NULL);

  light_source_changed (combo, config);

  /* Create overall layout */
  main_hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                              "main-hbox",
                                              "main-notebook",
                                              NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (main_hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_hbox), vbox, 0);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (appwin), "main-hbox", NULL);

  gtk_widget_set_visible (appwin, TRUE);

  {
    GdkCursor *cursor;

    cursor = gdk_cursor_new_for_display (gtk_widget_get_display (previewarea),
                                         GDK_HAND2);
    gdk_window_set_cursor (gtk_widget_get_window (previewarea), cursor);
    g_object_unref (cursor);
  }

  if (image_setup (drawable, TRUE))
    preview_compute ();

  g_signal_connect (combo, "value-changed",
                    G_CALLBACK (light_source_changed),
                    config);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (appwin));

  if (preview_rgb_data != NULL)
    g_free (preview_rgb_data);

  if (preview_surface != NULL)
    cairo_surface_destroy (preview_surface);

  gtk_widget_destroy (appwin);

  return run;
}


static void
save_lighting_preset (GtkWidget *widget,
                      gpointer   data)
{
  static GtkFileChooserNative *window = NULL;

  if (! window)
    {
      window =
        gtk_file_chooser_native_new (_("Save Lighting Preset"),
                                     GTK_WINDOW (appwin),
                                     GTK_FILE_CHOOSER_ACTION_SAVE,
                                     _("_Save"), _("_Cancel"));

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (window),
                                                      TRUE);

      g_signal_connect (window, "response",
                        G_CALLBACK (save_preset_response),
                        NULL);
    }

  if (lighting_effects_path)
    {
      GList *list;
      gchar *dir;

      list = clay_parse_search_path (lighting_effects_path, 256, FALSE, NULL);
      dir = clay_get_user_writable_dir (list);
      clay_free_paths (list);

      if (! dir)
        dir = g_strdup (clay_directory ());

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (window), dir);

      g_free (dir);
    }
  else
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (window),
                                           g_get_tmp_dir ());
    }

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (window));
}


static void
save_preset_response (GtkNativeDialog *chooser,
                      gint             response_id,
                      gpointer         data)
{
  FILE          *fp;
  gint           num_lights = 0;
  gint           k;
  LightSettings *source;
  gchar          buffer1[G_ASCII_DTOSTR_BUF_SIZE];
  gchar          buffer2[G_ASCII_DTOSTR_BUF_SIZE];
  gchar          buffer3[G_ASCII_DTOSTR_BUF_SIZE];
  gint           blen       = G_ASCII_DTOSTR_BUF_SIZE;

  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename =
        gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

      fp = g_fopen (filename, "wb");

      if (!fp)
        {
          g_message (_("Could not open '%s' for writing: %s"),
                     filename, g_strerror (errno));
        }
      else
        {
          for (k = 0; k < NUM_LIGHTS; k++)
            if (mapvals.lightsource[k].type != NO_LIGHT)
              ++num_lights;

          fprintf (fp, "Number of lights: %d\n", num_lights);

          for (k = 0; k < NUM_LIGHTS; k++)
            if (mapvals.lightsource[k].type != NO_LIGHT)
              {
                source = &mapvals.lightsource[k];

                switch (source->type)
                  {
                  case POINT_LIGHT:
                    fprintf (fp, "Type: Point\n");
                    break;
                  case DIRECTIONAL_LIGHT:
                    fprintf (fp, "Type: Directional\n");
                    break;
                  case SPOT_LIGHT:
                    fprintf (fp, "Type: Spot\n");
                    break;
                  default:
                    g_warning ("Unknown light type: %d",
                               mapvals.lightsource[k].type);
                    continue;
                  }

                fprintf (fp, "Position: %s %s %s\n",
                         g_ascii_dtostr (buffer1, blen, source->position.x),
                         g_ascii_dtostr (buffer2, blen, source->position.y),
                         g_ascii_dtostr (buffer3, blen, source->position.z));

                fprintf (fp, "Direction: %s %s %s\n",
                         g_ascii_dtostr (buffer1, blen, source->direction.x),
                         g_ascii_dtostr (buffer2, blen, source->direction.y),
                         g_ascii_dtostr (buffer3, blen, source->direction.z));

                fprintf (fp, "Color: %s %s %s\n",
                         g_ascii_dtostr (buffer1, blen, source->color[0]),
                         g_ascii_dtostr (buffer2, blen, source->color[1]),
                         g_ascii_dtostr (buffer3, blen, source->color[2]));

                fprintf (fp, "Intensity: %s\n",
                         g_ascii_dtostr (buffer1, blen, source->intensity));
              }

          fclose (fp);
        }

      g_free (filename);
    }

  gtk_native_dialog_hide (chooser);
}

static void
load_lighting_preset (GtkWidget *widget,
                      gpointer   data)
{
  static GtkFileChooserNative *window = NULL;

  if (! window)
    {
      window =
        gtk_file_chooser_native_new (_("Load Lighting Preset"),
                                     GTK_WINDOW (appwin),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     _("_Open"), _("_Cancel"));

      g_signal_connect (window, "response",
                        G_CALLBACK (load_preset_response),
                        NULL);
    }

  if (lighting_effects_path)
    {
      GList *list;
      gchar *dir;

      list = clay_parse_search_path (lighting_effects_path, 256, FALSE, NULL);
      dir = clay_get_user_writable_dir (list);
      clay_free_paths (list);

      if (! dir)
        dir = g_strdup (clay_directory ());

      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (window), dir);

      g_free (dir);
    }
  else
    {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (window),
                                           g_get_tmp_dir ());
    }

  gtk_native_dialog_show (GTK_NATIVE_DIALOG (window));
}


static void
load_preset_response (GtkNativeDialog *chooser,
                      gint             response_id,
                      gpointer         data)
{
  FILE          *fp;
  gint           num_lights = 0;
  gint           k;
  LightSettings *source;
  gchar          buffer1[G_ASCII_DTOSTR_BUF_SIZE];
  gchar          buffer2[G_ASCII_DTOSTR_BUF_SIZE];
  gchar          buffer3[G_ASCII_DTOSTR_BUF_SIZE];
  gchar          type_label[21];
  gchar         *endptr;
  gchar          fmt_str[32];

  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename =
        gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

      fp = g_fopen (filename, "rb");

      if (!fp)
        {
          g_message (_("Could not open '%s' for reading: %s"),
                     filename, g_strerror (errno));
        }
      else
        {
#ifndef _UCRT
          fscanf (fp, "Number of lights: %d", &num_lights);
#else
          fscanf_s (fp, "Number of lights: %d", &num_lights);
#endif

          /* clamp to what the array can hold, don't trust the file */
          num_lights = CLAMP (num_lights, 0, NUM_LIGHTS);

          /* initialize lights to off */
          for (k = 0; k < NUM_LIGHTS; k++)
            mapvals.lightsource[k].type = NO_LIGHT;

          for (k = 0; k < num_lights; k++)
            {
              source = &mapvals.lightsource[k];

#ifndef _UCRT
              fscanf (fp, " Type: %20s", type_label);
#else
              fscanf_s (fp, " Type: %20s", type_label, (unsigned int)sizeof(type_label));
#endif

              if (!strcmp (type_label, "Point"))
                source->type = POINT_LIGHT;
              else if (!strcmp (type_label, "Directional"))
                source->type = DIRECTIONAL_LIGHT;
              else if (!strcmp (type_label, "Spot"))
                source->type = SPOT_LIGHT;
              else
                {
                  g_warning ("Unknown light type: %s", type_label);
                  fclose (fp);
                  return;
                }

              snprintf (fmt_str, sizeof (fmt_str),
                        " Position: %%%" G_GSIZE_FORMAT "s %%%" G_GSIZE_FORMAT "s %%%" G_GSIZE_FORMAT "s",
                        sizeof (buffer1) - 1,
                        sizeof (buffer2) - 1,
                        sizeof (buffer3) - 1);
#ifndef _UCRT
              fscanf (fp, fmt_str, buffer1, buffer2, buffer3);
#else
              fscanf_s (fp, fmt_str,
                        buffer1, (unsigned int)sizeof(buffer1),
                        buffer2, (unsigned int)sizeof(buffer2),
                        buffer3, (unsigned int)sizeof(buffer3));
#endif
              source->position.x = g_ascii_strtod (buffer1, &endptr);
              source->position.y = g_ascii_strtod (buffer2, &endptr);
              source->position.z = g_ascii_strtod (buffer3, &endptr);

              snprintf (fmt_str, sizeof (fmt_str),
                        " Direction: %%%" G_GSIZE_FORMAT "s %%%" G_GSIZE_FORMAT "s %%%" G_GSIZE_FORMAT "s",
                        sizeof (buffer1) - 1,
                        sizeof (buffer2) - 1,
                        sizeof (buffer3) - 1);
#ifndef _UCRT
              fscanf (fp, fmt_str, buffer1, buffer2, buffer3);
#else
              fscanf_s (fp, fmt_str,
                        buffer1, (unsigned int)sizeof(buffer1),
                        buffer2, (unsigned int)sizeof(buffer2),
                        buffer3, (unsigned int)sizeof(buffer3));
#endif
              source->direction.x = g_ascii_strtod (buffer1, &endptr);
              source->direction.y = g_ascii_strtod (buffer2, &endptr);
              source->direction.z = g_ascii_strtod (buffer3, &endptr);

              snprintf (fmt_str, sizeof (fmt_str),
                        " Color: %%%" G_GSIZE_FORMAT "s %%%" G_GSIZE_FORMAT "s %%%" G_GSIZE_FORMAT "s",
                        sizeof (buffer1) - 1,
                        sizeof (buffer2) - 1,
                        sizeof (buffer3) - 1);
#ifndef _UCRT
              fscanf (fp, fmt_str, buffer1, buffer2, buffer3);
#else
              fscanf_s (fp, fmt_str,
                        buffer1, (unsigned int)sizeof(buffer1),
                        buffer2, (unsigned int)sizeof(buffer2),
                        buffer3, (unsigned int)sizeof(buffer3));
#endif
              source->color[0] = g_ascii_strtod (buffer1, &endptr);
              source->color[1] = g_ascii_strtod (buffer2, &endptr);
              source->color[2] = g_ascii_strtod (buffer3, &endptr);
              source->color[3] = 1.0;

              snprintf (fmt_str, sizeof (fmt_str),
                        " Intensity: %%%" G_GSIZE_FORMAT "s",
                        sizeof (buffer1) - 1);
#ifndef _UCRT
              fscanf (fp, fmt_str, buffer1);
#else
              fscanf_s (fp, fmt_str, buffer1, (unsigned int)sizeof(buffer1));
#endif
              source->intensity = g_ascii_strtod (buffer1, &endptr);

            }

          fclose (fp);
        }

      g_free (filename);
   }

  gtk_native_dialog_hide (chooser);
  interactive_preview_callback (NULL);
}

static void
light_source_changed (GtkWidget *widget,
                      gpointer   data)
{
  gint                 light_source;
  gboolean             isolate = FALSE;
  ClayProcedureConfig *config  = (ClayProcedureConfig *) data;

  light_source = clay_procedure_config_get_choice_id (config, "which_light") + 1;
  g_object_get (config,
                "isolate", &isolate,
                NULL);

  for (gint i = 1; i <= 6; i++)
    {
      GtkWidget *hbox;
      gchar     *temp_box;

      temp_box = g_strdup_printf ("light-hbox-%d", i);
      hbox = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (appwin),
                                               temp_box, G_TYPE_NONE);
      g_free (temp_box);

      if (i == light_source)
        gtk_widget_set_visible (hbox, TRUE);
      else
        gtk_widget_set_visible (hbox, FALSE);

      if (isolate)
        {
          if (i == light_source)
            mapvals.lightsource[i - 1].active = TRUE;
          else
            mapvals.lightsource[i - 1].active = FALSE;
        }
    }

  mapvals.light_selected = clay_procedure_config_get_choice_id (config, "which-light");
  apply_settings (config);
  if (previewarea != NULL && GTK_IS_WIDGET (previewarea))
    update_preview (config);
}

static void
isolate_selected_light (ClayProcedureConfig *config)
{
  gint     k;
  gboolean isolate;

  g_object_get (config,
                "isolate", &isolate,
                NULL);

  if (isolate)
    {
      for (k = 0; k < NUM_LIGHTS; k++)
        if (k == (mapvals.light_selected))
          mapvals.lightsource[k].active = TRUE;
        else
          mapvals.lightsource[k].active = FALSE;
    }
  else
    {
      for (k = 0; k < NUM_LIGHTS; k++)
        mapvals.lightsource[k].active = TRUE;
    }
}
