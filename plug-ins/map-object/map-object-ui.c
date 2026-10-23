/**************************************************************/
/* Dialog creation and updaters, callbacks and event-handlers */
/**************************************************************/

#include "config.h"

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "arcball.h"
#include "map-object-ui.h"
#include "map-object-icons.h"
#include "map-object-image.h"
#include "map-object-apply.h"
#include "map-object-preview.h"
#include "map-object-main.h"

#include "libclay/stdplugins-intl.h"


GtkWidget          *previewarea       = NULL;

static GtkWidget   *appwin            = NULL;
static GtkNotebook *options_note_book = NULL;

static GtkWidget *pointlightwid;
static GtkWidget *viewpointlightwid;
static GtkWidget *dirlightwid;

static GtkWidget *sphere_page    = NULL;
static GtkWidget *box_page       = NULL;
static GtkWidget *cylinder_page  = NULL;

static guint left_button_pressed = FALSE;
static guint light_hit           = FALSE;


static gint preview_events             (GtkWidget           *area,
                                        GdkEvent            *event,
                                        gpointer             data);

static void update_light_pos_entries   (ClayProcedureConfig *config);

static void update_preview             (ClayProcedureConfig *config);

static void toggle_update              (GtkWidget           *widget,
                                        gpointer             data);

static void lightmenu_callback         (ClayProcedureConfig *config);

static void preview_callback           (GtkWidget           *widget,
                                        gpointer             data);


/******************************************************/
/* Update angle & position (redraw grid if necessary) */
/******************************************************/

static void
update_preview (ClayProcedureConfig *config)
{
  copy_from_config (config);

  if (mapvals.livepreview)
    compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

static void
update_light_pos_entries (ClayProcedureConfig *config)
{
  g_object_set (config,
                "light-position-x", mapvals.lightsource.position.x,
                "light-position-y", mapvals.lightsource.position.y,
                "light-position-z", mapvals.lightsource.position.z,
                NULL);

  if (mapvals.livepreview)
    compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

/**********************/
/* Std. toggle update */
/**********************/

static void
toggle_update (GtkWidget *widget,
               gpointer   data)
{
  clay_toggle_button_update (widget, data);

  compute_preview_image ();
  gtk_widget_queue_draw (previewarea);
}

/*****************************************/
/* Main window light type menu callback. */
/*****************************************/

static void
lightmenu_callback (ClayProcedureConfig *config)
{
  gint light_type;

  light_type = clay_procedure_config_get_choice_id (config, "light-type");

  if (light_type == POINT_LIGHT)
    {
      gtk_widget_set_visible (dirlightwid, FALSE);
      gtk_widget_set_visible (pointlightwid, TRUE);
      gtk_widget_set_visible (viewpointlightwid, TRUE);
    }
  else if (light_type == DIRECTIONAL_LIGHT)
    {
      gtk_widget_set_visible (dirlightwid, TRUE);
      gtk_widget_set_visible (pointlightwid, FALSE);
      gtk_widget_set_visible (viewpointlightwid, FALSE);
    }
  else
    {
      gtk_widget_set_visible (dirlightwid, FALSE);
      gtk_widget_set_visible (pointlightwid, FALSE);
      gtk_widget_set_visible (viewpointlightwid, FALSE);
    }

  if (mapvals.livepreview)
    {
      copy_from_config (config);
      compute_preview_image ();
      gtk_widget_queue_draw (previewarea);
    }
}

/***************************************/
/* Main window map type menu callback. */
/***************************************/

static void
mapmenu_callback (GtkWidget *widget,
                  gpointer   data)
{
  int map_type;
  ClayProcedureConfig *config = (ClayProcedureConfig *) data;

  map_type = clay_procedure_config_get_choice_id (config, "map-type");

  if (mapvals.livepreview)
    {
      copy_from_config (config);
      compute_preview_image ();
      gtk_widget_queue_draw (previewarea);
    }

  gtk_widget_set_visible (sphere_page, FALSE);
  gtk_widget_set_visible (box_page, FALSE);
  gtk_widget_set_visible (cylinder_page, FALSE);

  if (map_type == MAP_SPHERE)
    gtk_widget_set_visible (sphere_page, TRUE);
  else if (map_type == MAP_BOX)
    gtk_widget_set_visible (box_page, TRUE);
  else if (map_type == MAP_CYLINDER)
    gtk_widget_set_visible (cylinder_page, TRUE);
}

/******************************************/
/* Main window "Preview!" button callback */
/******************************************/

static void
preview_callback (GtkWidget *widget,
                  gpointer   data)
{
  ClayProcedureConfig *config = (ClayProcedureConfig *) data;

  copy_from_config (config);
  compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

static void
zoomed_callback (ClayZoomModel *model)
{
  mapvals.zoom = clay_zoom_model_get_factor (model);

  compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

/******************************/
/* Preview area event handler */
/******************************/

static gint
preview_events (GtkWidget *area,
                GdkEvent  *event,
                gpointer   data)
{
  HVect __attribute__((unused))pos;
  ClayProcedureConfig *config = CLAY_PROCEDURE_CONFIG (data);
/*  HMatrix RotMat;
  gdouble a,b,c; */

  switch (event->type)
    {
      case GDK_ENTER_NOTIFY:
        break;

      case GDK_LEAVE_NOTIFY:
        break;

      case GDK_BUTTON_PRESS:
        light_hit = check_light_hit (event->button.x, event->button.y);
        if (light_hit == FALSE)
          {
            pos.x = -(2.0 * (gdouble) event->button.x /
                      (gdouble) PREVIEW_WIDTH - 1.0);
            pos.y = (2.0 * (gdouble) event->button.y /
                     (gdouble) PREVIEW_HEIGHT - 1.0);
            /*ArcBall_Mouse(pos);
            ArcBall_BeginDrag(); */
          }
        left_button_pressed = TRUE;
        break;

      case GDK_BUTTON_RELEASE:
        if (light_hit == TRUE)
          {
            compute_preview_image ();

            gtk_widget_queue_draw (previewarea);
          }
        else
          {
            pos.x = -(2.0 * (gdouble) event->button.x /
                      (gdouble) PREVIEW_WIDTH - 1.0);
            pos.y = (2.0 * (gdouble) event->button.y /
                     (gdouble) PREVIEW_HEIGHT - 1.0);
            /*ArcBall_Mouse(pos);
            ArcBall_EndDrag(); */
          }
        left_button_pressed = FALSE;
        break;

      case GDK_MOTION_NOTIFY:
        if (left_button_pressed == TRUE)
          {
            if (light_hit == TRUE)
              {
                gint live = mapvals.livepreview;

                mapvals.livepreview = FALSE;
                update_light (event->motion.x, event->motion.y);
                update_light_pos_entries (config);
                mapvals.livepreview = live;

                gtk_widget_queue_draw (previewarea);
              }
            else
              {
                pos.x = -(2.0 * (gdouble) event->motion.x /
                          (gdouble) PREVIEW_WIDTH - 1.0);
                pos.y = (2.0 * (gdouble) event->motion.y /
                         (gdouble) PREVIEW_HEIGHT - 1.0);
/*                ArcBall_Mouse(pos);
                ArcBall_Update();
                ArcBall_Values(&a,&b,&c);
                Alpha+=RadToDeg(-a);
                Beta+RadToDeg(-b);
                Gamma+=RadToDeg(-c);
                if (Alpha>180) Alpha-=360;
                if (Alpha<-180) Alpha+=360;
                if (Beta>180) Beta-=360;
                if (Beta<-180) Beta+=360;
                if (Gamma>180) Gamma-=360;
                if (Gamma<-180) Gamma+=360;
                      UpdateAngleSliders(); */
              }
          }
        break;

      default:
        break;
    }

  return FALSE;
}

/********************************/
/* Create and show main dialog. */
/********************************/

gboolean
main_dialog (ClayProcedure       *procedure,
             ClayProcedureConfig *config,
             ClayDrawable        *drawable)
{
  GtkWidget     *main_hbox;
  GtkWidget     *preview_box;
  GtkWidget     *hbox;
  GtkWidget     *frame;
  GtkWidget     *button;
  GtkWidget     *toggle;
  GtkWidget     *scale;
  GtkWidget     *spin;
  ClayZoomModel *model;
  GtkWidget     *map_combo;
  gboolean       run = FALSE;

  clay_ui_init (PLUG_IN_BINARY);

  appwin = clay_procedure_dialog_new (procedure,
                                      CLAY_PROCEDURE_CONFIG (config),
                                      _("Map to Object"));

  /* Create the Preview */

  preview_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_visible (preview_box, TRUE);

  /* Add preview widget and various buttons to the first part */

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (preview_box), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  gtk_widget_realize (appwin);
  previewarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (previewarea, PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (previewarea, (GDK_EXPOSURE_MASK |
                                       GDK_BUTTON1_MOTION_MASK |
                                       GDK_BUTTON_PRESS_MASK |
                                       GDK_BUTTON_RELEASE_MASK));
  gtk_container_add (GTK_CONTAINER (frame), previewarea);
  gtk_widget_set_visible (previewarea, TRUE);

  g_signal_connect (previewarea, "event",
                    G_CALLBACK (preview_events),
                    config);

  g_signal_connect (previewarea, "draw",
                    G_CALLBACK (preview_draw),
                    previewarea);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (preview_box), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  button = gtk_button_new_with_mnemonic (_("_Preview!"));
  g_object_set (gtk_bin_get_child (GTK_BIN (button)),
                "margin-start", 2,
                "margin-end",   2,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (preview_callback),
                    config);

  clay_help_set_help_data (button, _("Recompute preview image"), NULL);

  model = clay_zoom_model_new ();
  clay_zoom_model_set_range (model, 0.25, 1.0);
  clay_zoom_model_zoom (model, CLAY_ZOOM_TO, mapvals.zoom);

  button = clay_zoom_button_new (model, CLAY_ZOOM_IN, GTK_ICON_SIZE_MENU);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  button = clay_zoom_button_new (model, CLAY_ZOOM_OUT, GTK_ICON_SIZE_MENU);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_signal_connect (model, "zoomed",
                    G_CALLBACK (zoomed_callback),
                    NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("Show _wireframe"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mapvals.showgrid);
  gtk_box_pack_start (GTK_BOX (preview_box), toggle, FALSE, FALSE, 0);
  gtk_widget_set_visible (toggle, TRUE);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.showgrid);

  toggle = gtk_check_button_new_with_mnemonic (_("Update preview _live"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mapvals.livepreview);
  gtk_box_pack_start (GTK_BOX (preview_box), toggle, FALSE, FALSE, 0);
  gtk_widget_set_visible (toggle, TRUE);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.livepreview);

  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "options-tab", _("O_ptions"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "light-tab", _("Li_ght"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "viewpoint-tab", _("_Viewpoint"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "material-tab", _("_Material"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "orientation-tab", _("Orient_ation"),
                                   FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "sphere-tab", _("Sp_here"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "box-tab", _("_Box"), FALSE, TRUE);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "cylinder-tab", _("C_ylinder"), FALSE, TRUE);

  /* Options Tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "general-options", _("General Options"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "general-box",
                                  "map-type",
                                  "transparent-background",
                                  "tiled",
                                  "new-image",
                                  "new-layer",
                                  NULL);
  map_combo = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (appwin),
                                                "map-type", G_TYPE_NONE);
  g_signal_connect (map_combo, "value-changed",
                    G_CALLBACK (mapmenu_callback),
                    config);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "options-frame",
                                    "general-options", FALSE,
                                    "general-box");

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "antialias-box",
                                  "depth",
                                  "threshold",
                                  NULL);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "depth", 1.0);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "antialias-frame",
                                    "antialiasing", FALSE,
                                    "antialias-box");

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "options-box",
                                  "options-frame",
                                  "antialias-frame",
                                  NULL);

  g_signal_connect (config, "notify::transparent-background",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::tiled",
                    G_CALLBACK (update_preview),
                    config);

  /* Light Tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "light-settings-label", _("Light Settings"),
                                   FALSE, FALSE);

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "light-settings-box",
                                  "light-type",
                                  "light-color",
                                  NULL);
  g_signal_connect (config, "notify::light-type",
                    G_CALLBACK (lightmenu_callback),
                    config);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "light-frame",
                                    "light-settings-label", FALSE,
                                    "light-settings-box");

  /* Depending on light settings, only one of these are visible */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "vector-label", _("Direction Vector"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "vector-box",
                                  "light-direction-x",
                                  "light-direction-y",
                                  "light-direction-z",
                                  NULL);
  dirlightwid = clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                                  "vector-frame",
                                                  "vector-label", FALSE,
                                                  "vector-box");

  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "position-label", _("Position"),
                                   FALSE, FALSE);

  spin = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (appwin),
                                           "light-position-x",
                                           CLAY_TYPE_LABEL_SPIN);
  clay_label_spin_set_increments (CLAY_LABEL_SPIN (spin), 0.1, 0.1);
  spin = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (appwin),
                                           "light-position-y",
                                           CLAY_TYPE_LABEL_SPIN);
  clay_label_spin_set_increments (CLAY_LABEL_SPIN (spin), 0.1, 0.1);
  spin = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (appwin),
                                           "light-position-z",
                                           CLAY_TYPE_LABEL_SPIN);
  clay_label_spin_set_increments (CLAY_LABEL_SPIN (spin), 0.1, 0.1);

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "position-box",
                                  "light-position-x",
                                  "light-position-y",
                                  "light-position-z",
                                  NULL);
  pointlightwid = clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                                    "position-frame",
                                                    "position-label", FALSE,
                                                    "position-box");
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin), "light-box",
                                  "light-frame",
                                  "vector-frame",
                                  "position-frame",
                                  NULL);

  g_signal_connect (config, "notify::light-color",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-direction-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-direction-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-direction-z",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-position-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-position-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::light-position-z",
                    G_CALLBACK (update_preview),
                    config);

  /* Viewpoint Tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "viewpoint-position-label", _("Viewpoint Position"),
                                   FALSE, FALSE);
  hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                         "viewpoint-position-box",
                                         "viewpoint-x",
                                         "viewpoint-y",
                                         "viewpoint-z",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  viewpointlightwid = clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                                        "viewpoint-position-frame",
                                                        "viewpoint-position-label", FALSE,
                                                        "viewpoint-position-box");

  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "first-axis-x", 1.0);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "first-axis-y", 1.0);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "first-axis-z", 1.0);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "first-axis-label", _("First Axis"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "first-axis-box",
                                  "first-axis-x",
                                  "first-axis-y",
                                  "first-axis-z",
                                  NULL);
  frame = clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                            "first-axis-frame",
                                            "first-axis-label", FALSE,
                                            "first-axis-box");
  gtk_widget_set_hexpand (frame, TRUE);

  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "second-axis-x", 1.0);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "second-axis-y", 1.0);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "second-axis-z", 1.0);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "second-axis-label", _("Second Axis"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "second-axis-box",
                                  "second-axis-x",
                                  "second-axis-y",
                                  "second-axis-z",
                                  NULL);
  frame = clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                            "second-axis-frame",
                                            "second-axis-label", FALSE,
                                            "second-axis-box");
  gtk_widget_set_hexpand (frame, TRUE);

  hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                         "axis-box",
                                         "first-axis-frame",
                                         "second-axis-frame",
                                         NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "viewpoint-box",
                                  "viewpoint-position-frame",
                                  "axis-box",
                                  NULL);

  g_signal_connect (config, "notify::viewpoint-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::viewpoint-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::viewpoint-z",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::first-axis-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::first-axis-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::first-axis-z",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::second-axis-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::second-axis-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::second-axis-z",
                    G_CALLBACK (update_preview),
                    config);
  lightmenu_callback (config);

  /* Material Tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "intensity-label", _("Intensity Levels"),
                                   FALSE, FALSE);
  /* TODO: Restore icons */
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "intensity-box",
                                  "ambient-intensity",
                                  "diffuse-intensity",
                                  NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "intensity-frame",
                                    "intensity-label", FALSE,
                                    "intensity-box");

  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "reflexivity-label", _("Reflectivity"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "reflexivity-box",
                                  "diffuse-reflectivity",
                                  "specular-reflectivity",
                                  "highlight",
                                  NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "reflexivity-frame",
                                    "reflexivity-label", FALSE,
                                    "reflexivity-box");

   clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "material-box",
                                  "intensity-frame",
                                  "reflexivity-frame",
                                  NULL);

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

  /* Orientation Tab */
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "position-x", 1.0);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "position-y", 1.0);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "position-z", 1.0);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "orientation-position-label", _("Position"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "orientation-position-box",
                                  "position-x",
                                  "position-y",
                                  "position-z",
                                  NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "orientation-position-frame",
                                    "orientation-position-label", FALSE,
                                    "orientation-position-box");

  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "rotation-angle-x", 1.0);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "rotation-angle-y", 1.0);
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                        "rotation-angle-z", 1.0);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "rotation-angle-label", _("Rotation"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "rotation-angle-box",
                                  "rotation-angle-x",
                                  "rotation-angle-y",
                                  "rotation-angle-z",
                                  NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "rotation-angle-frame",
                                    "rotation-angle-label", FALSE,
                                    "rotation-angle-box");

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "orientation-box",
                                  "orientation-position-frame",
                                  "rotation-angle-frame",
                                  NULL);

  g_signal_connect (config, "notify::position-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::position-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::position-z",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::rotation-angle-x",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::rotation-angle-y",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::rotation-angle-z",
                    G_CALLBACK (update_preview),
                    config);

  /* Sphere Tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "sphere-label", _("Size"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "sphere-frame",
                                    "sphere-label", FALSE,
                                    "sphere-radius");

  g_signal_connect (config, "notify::sphere-radius",
                    G_CALLBACK (update_preview),
                    config);

  /* Box Tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "box-drawable-label", _("Map Images to Box Faces"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_flowbox (CLAY_PROCEDURE_DIALOG (appwin),
                                      "box-drawable-box",
                                      "box-front-drawable",
                                      "box-back-drawable",
                                      "box-top-drawable",
                                      "box-bottom-drawable",
                                      "box-left-drawable",
                                      "box-right-drawable",
                                      NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "box-drawable-frame",
                                    "box-drawable-label", FALSE,
                                    "box-drawable-box");

  scale = clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                                "x-scale", 1.0);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 0, 5.0);
  scale = clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                                "y-scale", 1.0);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 0, 5.0);
  scale = clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                                "z-scale", 1.0);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 0, 5.0);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "box-scale-box",
                                  "x-scale",
                                  "y-scale",
                                  "z-scale",
                                  NULL);

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "box-box",
                                  "box-drawable-frame",
                                  "box-scale-box",
                                  NULL);

  g_signal_connect (config, "notify::box-front-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-back-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-top-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-bottom-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-left-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::box-right-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::x-scale",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::y-scale",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::z-scale",
                    G_CALLBACK (update_preview),
                    config);

  /* Cylinder Tab */
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "cyl-drawable-label", _("Images for the Cap Faces"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                      "cyl-drawable-box",
                                      "cyl-top-drawable",
                                      "cyl-bottom-drawable",
                                      NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (appwin),
                                    "cyl-drawable-frame",
                                    "cyl-drawable-label", FALSE,
                                    "cyl-drawable-box");

  scale = clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                                "cylinder-radius", 1.0);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 0, 2.0);
  scale = clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (appwin),
                                                "cylinder-length", 1.0);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), 0, 2.0);
  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (appwin),
                                   "cyl-size-label", _("Size"),
                                   FALSE, FALSE);
  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "cyl-size-box",
                                  "cylinder-radius",
                                  "cylinder-length",
                                  NULL);

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                  "cylinder-box",
                                  "cyl-drawable-frame",
                                  "cyl-size-box",
                                  NULL);

  g_signal_connect (config, "notify::cyl-top-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::cyl-bottom-drawable",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::cylinder-radius",
                    G_CALLBACK (update_preview),
                    config);
  g_signal_connect (config, "notify::cylinder-length",
                    G_CALLBACK (update_preview),
                    config);

  options_note_book =
    GTK_NOTEBOOK (clay_procedure_dialog_fill_notebook (CLAY_PROCEDURE_DIALOG (appwin),
                                                       "main-notebook",
                                                       "options-tab", "options-box",
                                                       "light-tab", "light-box",
                                                       "viewpoint-tab", "viewpoint-box",
                                                       "orientation-tab", "orientation-box",
                                                       "material-tab", "material-box",
                                                       "sphere-tab", "sphere-frame",
                                                       "box-tab", "box-box",
                                                       "cylinder-tab", "cylinder-box",
                                                       NULL));

  /* Save reference to tabs for hiding/showing */
  sphere_page   = gtk_notebook_get_nth_page (options_note_book, 5);
  box_page      = gtk_notebook_get_nth_page (options_note_book, 6);
  cylinder_page = gtk_notebook_get_nth_page (options_note_book, 7);

  mapmenu_callback (map_combo, config);

  /* Create overall layout */
  main_hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (appwin),
                                              "main-hbox",
                                              "main-notebook",
                                              NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (main_hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (main_hbox), preview_box, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_hbox), preview_box, 0);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (appwin), "main-hbox", NULL);

  gtk_widget_set_visible (appwin, TRUE);

  {
    GdkCursor *cursor;

    cursor = gdk_cursor_new_for_display (gtk_widget_get_display (previewarea),
                                         GDK_HAND2);
    gdk_window_set_cursor (gtk_widget_get_window (previewarea), cursor);
    g_object_unref (cursor);
  }

  copy_from_config (config);
  image_setup (drawable, TRUE, config);

  compute_preview_image ();

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (appwin));

  gtk_widget_destroy (appwin);
  if (preview_rgb_data)
    g_free (preview_rgb_data);
  if (preview_surface)
    cairo_surface_destroy (preview_surface);

  return run;
}
